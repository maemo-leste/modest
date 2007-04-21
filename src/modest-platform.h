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

#ifndef __MODEST_PLATFORM_H__
#define __MODEST_PLATFORM_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <tny-device.h>

G_BEGIN_DECLS

/**
 * modest_platform_platform_init:
 *
 * platform specific initialization function
 * 
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean modest_platform_init (void);


/**
 * modest_platform_get_new_device:
 *
 * platform specific initialization function
 * 
 * Returns: TRUE if succeeded, FALSE otherwise
 */
TnyDevice*  modest_platform_get_new_device (void);


/**
 * modest_platform_get_file_icon_name:
 * @name: the name of the file, or NULL
 * @mime_type: the mime-type, or NULL
 * @effective_mime_type: out-param which receives the 'effective mime-type', ie., the mime type
 * that will be used. May be NULL if you're not interested in this. Note: the returned string
 * is newly allocated, and should be g_free'd when done with it.
 *
 * this function gets the icon for the file, based on the file name and/or the mime type,
 * using the following strategy:
 * (1) if mime_type != NULL and mime_type != application/octet-stream, find the
 *     the icon name for this mime type
 * (2) otherwise, guess the icon type from the file name, and then goto (1)
 *
 * Returns: the icon name
 */
gchar*  modest_platform_get_file_icon_name (const gchar* name, const gchar* mime_type,
					    gchar **effective_mime_type);

/**
 * modest_platform_activate_uri:
 * @uri: the uri to activate
 *
 * This function activates an URI
 *
 * Returns: %TRUE if successful, %FALSE if not.
 **/
gboolean modest_platform_activate_uri (const gchar *uri);

/**
 * modest_platform_show_uri_popup:
 * @uri: an URI with the string
 *
 * This function show the popup of actions for an URI
 *
 * Returns: %TRUE if successful, %FALSE if not.
 **/
gboolean modest_platform_show_uri_popup (const gchar *uri);

/**
 * modest_platform_get_icon:
 * @name: the name of the icon
 *
 * this function returns an icon, or NULL in case of error 
 */
GdkPixbuf* modest_platform_get_icon (const gchar *name);


/**
 * modest_platform_get_application_name:
 *
 * this function returns the name of the application. Do not modify.
 */
const gchar* modest_platform_get_app_name (void);

G_END_DECLS

#endif /* __MODEST_PLATFORM_UTILS_H__ */

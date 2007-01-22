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


/* modest-icon-factory.h */

#ifndef __MODEST_ICON_FACTORY_H__
#define __MODEST_ICON_FACTORY_H__

#include <gdk/gdkpixbuf.h>

/**
 * modest_icon_factory_get_icon:
 * @name: the filename of a certain icon
 *
 * Returns: a GdkPixBuf for this icon, or NULL in case of error
 * You should NOT unref or modify the pixbuf in any way
 */
GdkPixbuf* modest_icon_factory_get_icon (const gchar *name);

/**
 * modest_icon_factory_get_icon_at_size:
 * @name: the filename of a certain icon
 * @width: the desired width of the icon
 * @height: the desired height of the icon
 *
 * Returns: a GdkPixBuf for this icon, or NULL in case of error
 * You should NOT unref or modify the pixbuf in any way
 */
GdkPixbuf* modest_icon_factory_get_icon_at_size (const gchar *name, guint width, guint height);

/* FIXME */
#define modest_icon_factory_get_small_icon(n) modest_icon_factory_get_icon_at_size(n,16,16)
#define modest_icon_factory_get_big_icon(n)   modest_icon_factory_get_icon_at_size(n,24,24)


#endif /*__MODEST_ICON_FACTORY_H__ */

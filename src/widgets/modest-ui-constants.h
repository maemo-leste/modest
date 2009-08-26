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

#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-defines.h>
#endif

#ifndef __MODEST_MAEMO_UI_CONSTANTS_H__
#define __MODEST_MAEMO_UI_CONSTANTS_H__

/* These are based on an email on the maemo-developers mailing list from Dirk-Jan Binnema, 
 * title "RE: Standard widget spacing and padding?":
 */

#ifndef MODEST_TOOLKIT_GTK 
#define MODEST_MARGIN_NONE 0
#define MODEST_DIALOG_WINDOW_MIN_HEIGHT 172
#define MODEST_DIALOG_WINDOW_MAX_WIDTH 642
#define MODEST_DIALOG_WINDOW_MIN_WIDTH 172
#ifdef MODEST_TOOLKIT_HILDON2
#define MODEST_COMPACT_HEADER_BG "#404040"
#define MODEST_MARGIN_HALF HILDON_MARGIN_HALF
#define MODEST_MARGIN_DEFAULT HILDON_MARGIN_DEFAULT
#define MODEST_MARGIN_DOUBLE HILDON_MARGIN_DOUBLE
#define MODEST_MARGIN_TRIPLE HILDON_MARGIN_TRIPLE
#define MODEST_DIALOG_WINDOW_MAX_HEIGHT 345
#define MODEST_EDITABLE_SIZE (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH)
#else
#define MODEST_MARGIN_HALF 3
#define MODEST_MARGIN_DEFAULT 6
#define MODEST_MARGIN_DOUBLE 12
#define MODEST_MARGIN_TRIPLE 18
#define MODEST_DIALOG_WINDOW_MAX_HEIGHT 394
#endif
#else /* MODEST_TOOLKIT_GTK */
/* TODO: review this values with GNOME HIG */
#define MODEST_MARGIN_DEFAULT 6
#define MODEST_MARGIN_DOUBLE 12
#define MODEST_MARGIN_TRIPLE 18
#define MODEST_MARGIN_HALF 3
#define MODEST_MARGIN_NONE 0
#define MODEST_DIALOG_WINDOW_MAX_HEIGHT 394
#define MODEST_DIALOG_WINDOW_MIN_HEIGHT 172
#define MODEST_DIALOG_WINDOW_MAX_WIDTH 642
#define MODEST_DIALOG_WINDOW_MIN_WIDTH 172
#endif
#define MODEST_FOLDER_PATH_SEPARATOR " / "
#define MODEST_FOLDER_DOT " · "

#endif /*__MODEST_MAEMO_UI_CONSTANTS_H__*/

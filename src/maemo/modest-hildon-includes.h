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
k * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __MODEST_HILDON_INCLUDES__
#define __MODEST_HILDON_INCLUDES__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* helplib to use */
#ifdef MODEST_HAVE_OSSO_HELP
#include <osso-helplib.h>
#else
#ifdef MODEST_HAVE_HILDON_HELP
#include <hildon/hildon-help.h>
#endif /*MODEST_HAVE_HILDON_HELP*/
#endif /*MODEST_HAVE_OSSO_HELP*/

/* mimelib to use */
#ifdef MODEST_HAVE_OSSO_MIME
#include <osso-mime.h>
#include <osso-uri.h>
#else
#ifdef MODEST_HAVE_HILDON_MIME
#include <hildon-mime.h>
#include <hildon-uri.h>
#endif /*MODEST_HAVE_HILDON_MIME*/
#endif /*MODEST_HAVE_OSSO_MIME*/


#ifdef MODEST_HAVE_HILDON_NOTIFY
#include <hildon/hildon-notification.h>
#endif /*MODEST_HILDON_NOTIFY*/


#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <hildon-widgets/hildon-color-selector.h>
#include <hildon-widgets/hildon-color-button.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-number-editor.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-font-selection-dialog.h>
#include <hildon-widgets/hildon-find-toolbar.h>
#include <hildon-widgets/hildon-sort-dialog.h>

#else

#ifdef MODEST_HAVE_HILDON1_WIDGETS

#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-color-chooser.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-color-button.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-color-button.h>
#include <hildon/hildon-font-selection-dialog.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-find-toolbar.h>
#include <hildon/hildon-sort-dialog.h>
#include <hildon/hildon-number-editor.h>

#endif /*__MODEST_HAVE_HILDON1_WIDGETS*/
#endif /*__MODEST_HAVE_HILDON0_WIDGETS_*/

#endif /*__MODEST_HILDON_INCLUDES__*/

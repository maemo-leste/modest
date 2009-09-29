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

#if MODEST_HILDON_API >= 1
#include <hildon/hildon-helper.h>
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
#include <hildon/hildon-program.h>
#endif /* MODEST_HILDON_API >= 1 */


/* backward compatibility... */
#ifdef MODEST_HAVE_OSSO_MIME
#define hildon_mime_open_file_with_mime_type osso_mime_open_file_with_mime_type 
#define hildon_mime_open_file                osso_mime_open_file                

#define HildonURIAction                      OssoURIAction
#define hildon_uri_get_scheme_from_uri       osso_uri_get_scheme_from_uri
#define hildon_uri_get_actions               osso_uri_get_actions
#define hildon_uri_get_actions_by_uri        osso_uri_get_actions_by_uri
#define hildon_uri_action_get_translation    osso_uri_action_get_translation   
#define hildon_uri_is_default_action         osso_uri_is_default_action
#define hildon_uri_free_actions              osso_uri_free_actions

/* service->name */
#define hildon_uri_action_get_service        osso_uri_action_get_name
#define hildon_uri_open                      osso_uri_open

#define hildon_mime_get_icon_names           osso_mime_get_icon_names 
#endif /*MODEST_HAVE_OSSO_MIME*/

/* helplib to use */
#ifdef MODEST_HAVE_OSSO_HELP
#define hildon_help_show               ossohelp_show
#define hildon_help_dialog_help_enable ossohelp_dialog_help_enable
#define HILDON_HELP_SHOW_DIALOG        OSSO_HELP_SHOW_DIALOG
#else
#ifdef MODEST_HAVE_HILDON_HELP
/* nothing */
#endif /*MODEST_HAVE_HILDON_HELP*/
#endif /*MODEST_HAVE_OSSO_HELP*/

/* some extra #defines, so it will compile with the 'normal' gtk */
#ifndef MODEST_HAVE_HILDON_GTK
#define hildon_gtk_entry_set_input_mode(a,b) \
	g_debug ("%s: hildon_gtk_entry_set_input_mode requires gtk-hildon", __FUNCTION__)
#define hildon_gtk_text_view_set_input_mode(a,b) \
	g_debug ("%s: hildon_gtk_text_view_set_input_mode requires gtk-hildon", __FUNCTION__)
#define gtk_widget_tap_and_hold_setup(a,b,c,d)				\
	g_debug ("%s: gtk_widget_tap_and_hold_setup requires gtk-hildon", __FUNCTION__)

typedef enum
{
  GTK_INVALID_INPUT_MAX_CHARS_REACHED,
  GTK_INVALID_INPUT_MODE_RESTRICTION
} GtkInvalidInputType;

typedef enum
{
  HILDON_GTK_INPUT_MODE_ALPHA        = 1 << 0,
  HILDON_GTK_INPUT_MODE_NUMERIC      = 1 << 1,
  HILDON_GTK_INPUT_MODE_SPECIAL      = 1 << 2,
  HILDON_GTK_INPUT_MODE_HEXA         = 1 << 3,
  HILDON_GTK_INPUT_MODE_TELE         = 1 << 4,
  HILDON_GTK_INPUT_MODE_FULL         = (HILDON_GTK_INPUT_MODE_ALPHA | HILDON_GTK_INPUT_MODE_NUMERIC | HILDON_GTK_INPUT_MODE_SPECIAL),
  HILDON_GTK_INPUT_MODE_MULTILINE    = 1 << 28,
  HILDON_GTK_INPUT_MODE_INVISIBLE    = 1 << 29,
  HILDON_GTK_INPUT_MODE_AUTOCAP      = 1 << 30,
  HILDON_GTK_INPUT_MODE_DICTIONARY   = 1 << 31
} HildonGtkInputMode;
#endif /* !MODEST_HAVE_HILDON_GTK */

#endif /*__MODEST_HILDON_INCLUDES__*/

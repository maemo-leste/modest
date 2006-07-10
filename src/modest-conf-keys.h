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


#ifndef __MODEST_CONF_KEYS_H__
#define __MODEST_CONF_KEYS_H__

/* configuration key definitions for modest */
#define MODEST_CONF_NAMESPACE		"/apps/modest"

#define MODEST_CONF_USE_EXT_EDITOR	  MODEST_CONF_NAMESPACE "/use_ext_editor"	  /* boolean */
#define MODEST_CONF_EXT_EDITOR	          MODEST_CONF_NAMESPACE "/ext_editor"	  /* string */

#define MODEST_CONF_MAIN_WINDOW_HEIGHT	  MODEST_CONF_NAMESPACE "/main_window_height"    /* int */
#define MODEST_CONF_MAIN_WINDOW_HEIGHT_DEFAULT 480                                       /* int */

#define MODEST_CONF_MAIN_WINDOW_WIDTH	  MODEST_CONF_NAMESPACE "/main_window_width"     /* int */
#define MODEST_CONF_MAIN_WINDOW_WIDTH_DEFAULT  800

#define MODEST_CONF_EDIT_WINDOW_HEIGHT	  MODEST_CONF_NAMESPACE "/edit_window_height"    /* int */
#define MODEST_CONF_EDIT_WINDOW_HEIGHT_DEFAULT 480                                       /* int */

#define MODEST_CONF_EDIT_WINDOW_WIDTH	  MODEST_CONF_NAMESPACE "/edit_window_width"     /* int */
#define MODEST_CONF_EDIT_WINDOW_WIDTH_DEFAULT  800

#define MODEST_CONF_MSG_VIEW_NAMESPACE    MODEST_CONF_NAMESPACE "/view"

#define MODEST_CONF_MSG_VIEW_SHOW_ATTACHMENTS_INLINE MODEST_CONF_MSG_VIEW_NAMESPACE "/show_attachments_inline" /* boolean */


#endif /*__MODEST_CONF_KEYS_H__*/
